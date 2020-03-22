#!/usr/bin/env python

#
#    Copyright (c) 2017-2018 Nest Labs, Inc.
#    Copyright (c) 2019-2020 Google, LLC.
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
#       Acts a wrapper for WeaveDeviceMgr.py
#

import random
import os
import sys
import optparse
from optparse import OptionParser, Option, OptionValueError
from copy import copy
import shlex
import base64
import json
import set_test_path
import logging

weaveDeviceMgrPath = os.environ['WEAVE_DEVICE_MGR_PATH']
print 'weaveDeviceMgrPath is %s' % weaveDeviceMgrPath

if os.path.exists(weaveDeviceMgrPath):
    sys.path.append(weaveDeviceMgrPath)

try:
    from openweave import WeaveStack
except Exception as ex:
    print ("Failed to import WeaveStack: %s" % (str(ex)))

try:
    from openweave import WeaveDeviceMgr
except Exception as ex:
    print ("Failed to import WeaveDeviceMgr: %s" % (str(ex)))

try:
    from openweave import WdmClient
except Exception as ex:
    print ("Failed to import WdmClient: %s" % (str(ex)))

try:
    from openweave import GenericTraitUpdatableDataSink
except Exception as ex:
    print ("Failed to import GenericTraitUpdatableDataSink: %s" % (str(ex)))

try:
    from openweave import ResourceIdentifier
except Exception as ex:
    print ("Failed to import ResourceIdentifier: %s" % (str(ex)))

# Dummy Access Token
#
# The following fabric access token contains the dummy account certificate and
# private key described above.  This can be used to authenticate to the mock device
# when it has been configured to use the dummy service config.
#
dummyAccessToken = (
    "1QAABAAJADUBMAEITi8yS0HXOtskAgQ3AyyBEERVTU1ZLUFDQ09VTlQtSUQYJgTLqPobJgVLNU9C" +
    "NwYsgRBEVU1NWS1BQ0NPVU5ULUlEGCQHAiYIJQBaIzAKOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZ" +
    "TksL837axemzNfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4DWDKQEYNYIpASQCBRg1hCkBNgIEAgQB" +
    "GBg1gTACCEI8lV9GHlLbGDWAMAIIQjyVX0YeUtsYNQwwAR0AimGGYj0XstLP0m05PeQlaeCR6gVq" +
    "dc7dReuDzzACHHS0K6RtFGW3t3GaWq9k0ohgbrOxoDHKkm/K8kMYGDUCJgElAFojMAIcuvzjT4a/" +
    "fDgScCv5oxC/T5vz7zAPpURNQjpnajADOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZTksL837axemz" +
    "NfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4BgY"
)


DUMMY_WHERE_ID = '00000000-0000-0000-0000-000100000010'

DUMMY_SOPKEN_WHERE_ID = '00000000-0000-0000-0000-000100000010'

DUMMY_PAIRING_TOKEN = 'wu.WSmLH13lliU5zigrDm97vMjCBZUfHaFKyADQ/zRIkFjYjAJqrowWPkem7BrLXFg+HXz/iLIVj3TUHnRnZR2oNwZ2GfE='

DUMMY_SERVICE_CONFIG = '1QAADwABADYBFTABCQCoNCLp2XXkVSQCBDcDJxMBAADu7jC0GBgmBJUjqRkmBRXB0iw3BicTAQAA7u4wtBgYJAcCJggVAFojMAoxBHhS4pySunAZWEZtrhhySvtDDfYHKTMNYVXlZUaOug2lP7UXwEdkRAIYT6gRJFDUezWDKQEpAhg1gikBJAJgGDWBMAIIQgys9rRkceYYNYAwAghCDKz2tGRx5hg1DDABGQC+DtqhY1qO8VIXRYC93JQS1MwcLDNOKdwwAhkAi+fuLhEXFK6S2is7bS/XXZ5fzbi6L2V2GBgVMAEISOhQ0KHHNVskAgQ3AywBCDIxMjQ1NTMwGCYEBmDKHiYFBgnwMTcGLAEIMjEyNDU1MzAYJAcCJgglAFojMAo5BO2ka2bbHvtdCvtFIlELBnMxQYPbwKhJDE4FTZniehBCcYYhx+Z7ri5ncS22cjquARSKRCtNiANBNYMpARg1gikBJAIFGDWEKQE2AgQCBAEYGDWBMAIISBQYVpi13ycYNYAwAghIFBhWmLXfJxg1DDABHEmDxQmEkb6YJ+IYd2YLLxiYmxVH4pmFQXEd31MwAh0Ax2vFnMwPHZtC83XtjuAhofJJsbY99BXA+0lzFxgYGDUCJwEBAAAAAjC0GDYCFSwBImZyb250ZG9vci5pbnRlZ3JhdGlvbi5uZXN0bGFicy5jb20lAlcrGBgYGA=='

DUMMY_INIT_DATA = '{where_id: 00000000-0000-0000-0000-000100000010, structure_id: 8ae7af80-f152-11e5-ae52-22000b1905e2, spoken_where_id: 00000000-0000-0000-0000-000100000010}'


def DecodeBase64Option(option, opt, value):
    try:
        return base64.standard_b64decode(value)
    except TypeError:
        raise OptionValueError(
            "option %s: invalid base64 value: %r" % (opt, value))


def DecodeHexIntOption(option, opt, value):
    try:
        return int(value, 16)
    except ValueError:
        raise OptionValueError(
            "option %s: invalid value: %r" % (opt, value))

def parseNetworkId(value):
    if (value == '$last-network-id' or value == '$last'):
        if (lastNetworkId != None):
            return lastNetworkId
        else:
            print "No last network id"
            return None

    try:
        return int(value)
    except ValueError:
        print "Invalid network id"
        return None

class MockWeaveDataManagementClientImp():
    def __init_(self):
        self.wdmClient = None

    def createWdmClient(self):
        try:
            self.wdmClient = WdmClient.WdmClient()
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def closeWdmClient(self):
        try:
            if self.wdmClient:
                self.wdmClient.close()
                self.wdmClient = None
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

        print "close wdm client complete"

    def setNodeId(self, val):
        try:
            if self.wdmClient:
                self.wdmClient.setNodeId(val)
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return
        print "close wdm client complete"

    def newDataSink(self, statusProfileId, instanceid, path, testResOption="default"):
        """
          new-data-sink <statusProfileId, instanceid, path>
        """
        if self.wdmClient == None:
            print "wdmclient not initialized"
            return

        if testResOption == "default" :
            resourceIdentifier = ResourceIdentifier.ResourceIdentifier()
        elif testResOption == "ResTypeIdInt":
            resourceIdentifier = ResourceIdentifier.ResourceIdentifier.MakeResTypeIdInt(0, -2)
        elif testResOption == "ResTypeIdBytes":
            resourceIdentifier = ResourceIdentifier.ResourceIdentifier.MakeResTypeIdBytes(0, b'\xfe\xff\xff\xff\xff\xff\xff\xff')

        try:
            traitInstance = self.wdmClient.newDataSink(resourceIdentifier, statusProfileId, instanceid, path)
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

        print "new data sink complete"
        return traitInstance

    def flushUpdate(self):
        if self.wdmClient == None:
            print "wdmclient not initialized"
            return

        try:
            result = self.wdmClient.flushUpdate()
            print "flushUpdate Complete"
            print result
            return result
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def refreshData(self):
        if self.wdmClient == None:
            print "wdmclient not initialized"
            return

        try:
            result = self.wdmClient.refreshData()
            print "refresh trait data complete"
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def refreshIndividualData(self, traitInstance):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        try:
            result = traitInstance.refreshData()
            print "refresh individual trait data complete"
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def closeIndividualTrait(self, traitInstance):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        try:
            traitInstance.close()
            print "close current trait"
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def setData(self, traitInstance, path, value):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        isConditional = False

        try:
            val = ast.literal_eval(args[1])
        except:
            val = value

        try:
            traitInstance.setData(path, val, isConditional)
            print "set string data in trait complete"
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def getData(self, traitInstance, path):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        try:
            val = traitInstance.getData(path)
            print "get data in trait complete"
            print val
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def getVersion(self, traitInstance):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        try:
            val = traitInstance.getVersion()
            print val
            return val
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def clearTrait(self, traitInstance):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        try:
            traitInstance.clear()
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

    def deleteData(self, traitInstance, path):
        if self.wdmClient == None or traitInstance == None:
            print "wdmclient or traitInstance not initialized"
            return

        try:
            traitInstance.deleteData(path)
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            return

def testWdmClientCreateClose(testObject):
    testObject.createWdmClient()
    testObject.closeWdmClient()
    print "testWdmClientCreationClose completes"

def testWdmClientDataSinkCreateClose(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.closeWdmClient()
    print "testWdmClientDataSinkCreateClose completes"

def testWdmClientDataSinkEmptyFlushData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    results = testObject.flushUpdate()
    testObject.closeWdmClient()
    print "testWdmClientDataSinkEmptyFlushData completes"

def testWdmClientDataSinkSetFlushData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)
    results = testObject.flushUpdate()
    testObject.closeWdmClient()
    print "testWdmClientDataSinkSetFlushData completes"

def testWdmClientDataSinkSetFlushInvalidData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.setData(localeSettingsTrait, "/1", 0.1)
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", "TESTTEST")
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)
    results1 = testObject.flushUpdate()
    if len(results1) != 2:
        raise ValueError("testWdmClientDataSinkSetFlushInvalidData stage1  fails")
    print "testWdmClientDataSinkSetFlushInvalidData stage1 completes"

    results2 = testObject.flushUpdate()
    if len(results2) != 0:
        raise ValueError("testWdmClientDataSinkSetFlushInvalidData stage2  fails")
    print "testWdmClientDataSinkSetFlushInvalidData stage2 completes"

    for ele in results1:
        print "clear trait: " + str(ele.dataSink.profileId)
        print "clear trait path:" + str(ele.path)
        testObject.deleteData(ele.dataSink, ele.path)

    results3 = testObject.flushUpdate()
    if len(results3) != 0:
        raise ValueError("testWdmClientDataSinkSetFlushInvalidData stage3  fails")

    testObject.setData(TestCTrait, "/4", 20)
    results4 = testObject.flushUpdate()
    if len(results4) != 0:
        raise ValueError("testWdmClientDataSinkSetFlushInvalidData stage4  fails")

    testObject.closeWdmClient()
    print "testWdmClientDataSinkSetFlushInvalidData completes"

def testWdmClientDataSinkSetFlushDataDeleteData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)

    testObject.deleteData(localeSettingsTrait, "/1")
    testObject.deleteData(TestCTrait, "/1")
    testObject.deleteData(TestCTrait, "/2")
    testObject.deleteData(TestCTrait, "/3/1")
    testObject.deleteData(TestCTrait, "/3/2")

    results = testObject.flushUpdate()
    testObject.closeWdmClient()
    print "testWdmClientDataSinkSetFlushDataDeleteData completes"

def testWdmClientDataSinkSetClearFlushData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)

    testObject.clearTrait(localeSettingsTrait)
    testObject.clearTrait(TestCTrait)

    results = testObject.flushUpdate()
    testObject.closeWdmClient()
    print "testWdmClientDataSinkSetClearFlushData completes"

def testWdmClientDataSinkRefreshIndividualGetDataRefresh(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.refreshIndividualData(localeSettingsTrait)
    testObject.getData(localeSettingsTrait, "/1")
    testObject.refreshIndividualData(TestCTrait)
    TestCTraitVersion = testObject.getVersion(TestCTrait)
    testObject.getData(TestCTrait, "/1")
    testObject.getData(TestCTrait, "/2")
    testObject.getData(TestCTrait, "/3/1")
    testObject.getData(TestCTrait, "/3/2")
    testObject.getData(TestCTrait, "/4")
    testObject.refreshIndividualData(TestCTrait)
    TestCTraitVersion = testObject.getVersion(TestCTrait)
    testObject.closeWdmClient()
    print "testWdmClientDataSinkRefreshIndividualGetDataRefresh completes"

def testWdmClientDataSinkRefreshGetDataRefresh(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    refreshResults1 = testObject.refreshData()

    localeSettingsTraitVersion = testObject.getVersion(localeSettingsTrait)
    TestCTraitVersion = testObject.getVersion(TestCTrait)
    testObject.getData(localeSettingsTrait, "/1")
    testObject.getData(TestCTrait, "/1")
    testObject.getData(TestCTrait, "/2")
    testObject.getData(TestCTrait, "/3/1")
    testObject.getData(TestCTrait, "/3/2")
    testObject.getData(TestCTrait, "/4")

    refreshResults2 = testObject.refreshData()

    localeSettingsTraitVersion = testObject.getVersion(localeSettingsTrait)
    TestCTraitVersion = testObject.getVersion(TestCTrait)
    testObject.closeWdmClient()
    print "testWdmClientDataSinkRefreshGetDataRefresh completes"

def testWdmClientDataSinkCloseIndividualData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    refreshResults1 = testObject.refreshIndividualData(localeSettingsTrait)
    testObject.closeIndividualTrait(localeSettingsTrait)
    refreshResults2 = testObject.refreshIndividualData(TestCTrait)
    testObject.closeIndividualTrait(TestCTrait)
    testObject.closeWdmClient()
    print "testWdmClientDataSinkCloseIndividualData completes"

def testWdmClientDataSinkSetFlushRefreshGetData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    localeCapabilitiesTrait = testObject.newDataSink(21, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(localeCapabilitiesTrait, "/2", ["en-US", "zh-TW"])
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)
    testObject.flushUpdate()
    testObject.refreshData()
    testObject.getData(localeSettingsTrait, "/1")
    testObject.getData(localeCapabilitiesTrait, "/2")
    testObject.getData(TestCTrait, "/1")
    testObject.getData(TestCTrait, "/2")
    testObject.getData(TestCTrait, "/3/1")
    testObject.getData(TestCTrait, "/3/2")
    testObject.getData(TestCTrait, "/4")
    testObject.closeWdmClient()
    print "testWdmClientDataSinkSetFlushRefreshGetData completes"

def testWdmClientDataSinkSetRefreshFlushGetData(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/")
    TestCTrait = testObject.newDataSink(593165827, 0, "/")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)
    testObject.refreshData()
    testObject.flushUpdate()
    testObject.getData(localeSettingsTrait, "/1")
    testObject.getData(TestCTrait, "/1")
    testObject.getData(TestCTrait, "/2")
    testObject.getData(TestCTrait, "/3/1")
    testObject.getData(TestCTrait, "/3/2")
    testObject.getData(TestCTrait, "/4")
    testObject.closeWdmClient()
    print "testWdmClientDataSinkSetRefreshFlushGetData completes"

def testWdmClientDataSinkResourceIdentifierMakeResTypeIdInt(testObject):
    testObject.createWdmClient()
    localeSettingsTrait = testObject.newDataSink(20, 0, "/", "ResTypeIdInt")
    TestCTrait = testObject.newDataSink(593165827, 0, "/", "ResTypeIdInt")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)
    testObject.flushUpdate()
    testObject.refreshData()
    testObject.getData(localeSettingsTrait, "/1")
    testObject.getData(TestCTrait, "/1")
    testObject.getData(TestCTrait, "/2")
    testObject.getData(TestCTrait, "/3/1")
    testObject.getData(TestCTrait, "/3/2")
    testObject.getData(TestCTrait, "/4")
    testObject.closeWdmClient()
    print "testWdmClientDataSinkResourceIdentifierMakeResTypeIdInt completes"

def testWdmClientDataSinkResourceIdentifierMakeResTypeIdBytes(testObject):
    testObject.createWdmClient()
    testObject.setNodeId(-2)
    localeSettingsTrait = testObject.newDataSink(20, 0, "/", "ResTypeIdBytes")
    TestCTrait = testObject.newDataSink(593165827, 0, "/", "ResTypeIdBytes")
    testObject.setData(localeSettingsTrait, "/1", "en-US")
    testObject.setData(TestCTrait, "/1", False)
    testObject.setData(TestCTrait, "/2", 15)
    testObject.setData(TestCTrait, "/3/1", 16)
    testObject.setData(TestCTrait, "/3/2", False)
    testObject.setData(TestCTrait, "/4", 17)
    testObject.flushUpdate()
    testObject.refreshData()
    testObject.getData(localeSettingsTrait, "/1")
    testObject.getData(TestCTrait, "/1")
    testObject.getData(TestCTrait, "/2")
    testObject.getData(TestCTrait, "/3/1")
    testObject.getData(TestCTrait, "/3/2")
    testObject.getData(TestCTrait, "/4")
    testObject.closeWdmClient()
    print "testWdmClientDataSinkResourceIdentifierMakeResTypeIdBytes completes"

def RunWdmClientTest():
    print "Run Weave Data Management Test"
    testObject = MockWeaveDataManagementClientImp()
    testWdmClientCreateClose(testObject)
    testWdmClientDataSinkEmptyFlushData(testObject)
    testWdmClientDataSinkCreateClose(testObject)
    testWdmClientDataSinkSetFlushData(testObject)
    testWdmClientDataSinkSetFlushInvalidData(testObject)
    testWdmClientDataSinkSetFlushDataDeleteData(testObject)
    testWdmClientDataSinkSetClearFlushData(testObject)
    testWdmClientDataSinkSetFlushRefreshGetData(testObject)
    testWdmClientDataSinkRefreshGetDataRefresh(testObject)
    testWdmClientDataSinkRefreshIndividualGetDataRefresh(testObject)
    testWdmClientDataSinkCloseIndividualData(testObject)
    testWdmClientDataSinkSetRefreshFlushGetData(testObject)
    testWdmClientDataSinkResourceIdentifierMakeResTypeIdInt(testObject)
    testWdmClientDataSinkResourceIdentifierMakeResTypeIdBytes(testObject)

    print "Run Weave Data Management Complete"

class ExtendedOption (Option):
    TYPES = Option.TYPES + ("base64", "hexint", )
    TYPE_CHECKER = copy(Option.TYPE_CHECKER)
    TYPE_CHECKER["base64"] = DecodeBase64Option
    TYPE_CHECKER["hexint"] = DecodeHexIntOption


if __name__ == '__main__':
    
    # Override the default logging for WeaveStack to include timestamps.
    weaveStackLogger = logging.getLogger('openweave.WeaveStack')
    logHandler = logging.StreamHandler(stream=sys.stdout)
    logHandler.setFormatter(WeaveStack.WeaveLogFormatter(logTimestamp=True))
    weaveStackLogger.addHandler(logHandler)
    weaveStackLogger.setLevel(logging.DEBUG)

    devMgr = WeaveDeviceMgr.WeaveDeviceManager()

    cmdLine = " ".join(sys.argv[1:])
    args = shlex.split(cmdLine)

    optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)

    print "Connecting to device ..."


    print ''
    print '#################################connect#################################'
    optParser.add_option("", "--pairing-code", action="store", dest="pairingCode", type="string")
    optParser.add_option("", "--access-token", action="store", dest="accessToken", type="base64")
    optParser.add_option("", "--use-dummy-access-token", action="store_true", dest="useDummyAccessToken")
    optParser.add_option("", "--ble", action="store_true", dest="useBle")
    optParser.add_option("", "--account-id", action="store", dest="account_id")
    optParser.add_option("", "--service-config", action="store", dest="service_config")
    optParser.add_option("", "--pairing-token", action="store", dest="pairing_token")
    optParser.add_option("", "--init-data", action="store", dest="init_data")
    optParser.add_option("", "--ble-src-addr", action="store", dest="bleSrcAddr")
    optParser.add_option("", "--ble-dst-addr", action="store", dest="bleDstAddr")
    try:
        (options, remainingArgs) = optParser.parse_args(args)
    except SystemExit:
        exit()

    if (len(remainingArgs) > 2):
        print "Unexpected argument: " + remainingArgs[2]
        exit()

    if not options.useBle:
        addr = remainingArgs[0]
        remainingArgs.pop(0)

    if (len(remainingArgs)):
        nodeId = int(remainingArgs[0], 16)
        remainingArgs.pop(0)
    else:
        nodeId = 1

    if (options.useDummyAccessToken and not options.accessToken):
        options.accessToken = base64.standard_b64decode(dummyAccessToken)
    if (options.pairingCode and options.accessToken):
        print "Cannot specify both pairing code and access token"
        exit()

    try:
        if options.useBle:
            from openweave.WeaveBluezMgr import BluezManager as BleManager
            from openweave.WeaveBleUtility import FAKE_CONN_OBJ_VALUE
            bleManager = BleManager(devMgr)

            try:
                bleManager.ble_adapter_select(identifier=options.bleSrcAddr)
            except WeaveStack.WeaveStackException, ex:
                print ex
                exit()

            try:
                line = ' ' + str(options.bleDstAddr)
                bleManager.scan_connect(line)
            except WeaveStack.WeaveStackException, ex:
                print ex
                exit()

            devMgr.ConnectBle(bleConnection=FAKE_CONN_OBJ_VALUE,
                                   pairingCode=options.pairingCode,
                                   accessToken=options.accessToken)

            RunWdmClientTest()
            try:
                devMgr.Close()
                devMgr.CloseEndpoints()
                bleManager.disconnect()
            except WeaveStack.WeaveStackException, ex:
                print str(ex)
                exit()

            print "WoBLE central is good to go"
            print "Shutdown complete"
            exit()

        else:
            devMgr.ConnectDevice(deviceId=nodeId, deviceAddr=addr,
                                      pairingCode=options.pairingCode,
                                      accessToken=options.accessToken)

    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()


    if options.account_id is not None and options.service_config is not None and options.init_data is not None and options.pairing_token is not None:
        print ''
        print '#################################register-real-NestService#################################'
        try:
            devMgr.RegisterServicePairAccount(0x18B4300200000010, options.account_id, base64.b64decode(options.service_config), options.pairing_token, options.init_data)
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            exit()

        """
        #Disable unregister since the service automatically removes devices from accounts when they are paired again
        print "Register NestService done"

        print ''
        print '#################################unregister-real-NestService#################################'
        try:
            devMgr.UnregisterService(0x18B4300200000010)
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            exit()

        print "Unregister service done"
        """
        print ''
        print '#################################close#################################'

        try:
            devMgr.Close()
            devMgr.CloseEndpoints()
        except WeaveStack.WeaveStackException, ex:
            print str(ex)
            exit()
        print "Shutdown complete"
        exit()

    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################connect2#################################'
    try:
        devMgr.ConnectDevice(deviceId=nodeId, deviceAddr=addr)

    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()


    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################set-rendezvous-linklocal#################################'
    try:
        devMgr.SetRendezvousLinkLocal(0)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Done."

    print '#################################set-rendezvous-address#################################'
    try:
        devMgr.SetRendezvousAddress(addr)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Done."

    print ''
    print '#################################set-connect-timeout#################################'
    timeoutMS = 1000

    try:
        devMgr.SetConnectTimeout(timeoutMS)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Done."

    print ''
    print '#################################rendezvous1#################################'
    try:
        devMgr.RendezvousDevice(
                                accessToken=options.accessToken,
                                targetFabricId=WeaveDeviceMgr.TargetFabricId_AnyFabric,
                                targetModes=WeaveDeviceMgr.TargetDeviceMode_Any,
                                targetVendorId=WeaveDeviceMgr.TargetVendorId_Any,
                                targetProductId=WeaveDeviceMgr.TargetProductId_Any,
                                targetDeviceId=WeaveDeviceMgr.TargetDeviceId_Any)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Connected to device %X at %s" % (nodeId, addr)

    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################rendezvous2#################################'
    try:
        devMgr.RendezvousDevice(
                                pairingCode=options.pairingCode,
                                targetFabricId=WeaveDeviceMgr.TargetFabricId_AnyFabric,
                                targetModes=WeaveDeviceMgr.TargetDeviceMode_Any,
                                targetVendorId=WeaveDeviceMgr.TargetVendorId_Any,
                                targetProductId=WeaveDeviceMgr.TargetProductId_Any,
                                targetDeviceId=WeaveDeviceMgr.TargetDeviceId_Any)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Connected to device %X at %s" % (nodeId, addr)

    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################rendezvous3#################################'
    try:
        devMgr.RendezvousDevice(
                                pairingCode=options.pairingCode)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Connected to device %X at %s" % (nodeId, addr)

    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################rendezvous4#################################'
    try:
        devMgr.RendezvousDevice()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Connected to device %X at %s" % (nodeId, addr)

    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################connect#################################'
    try:
        devMgr.ConnectDevice(deviceId=nodeId, deviceAddr=addr, pairingCode=options.pairingCode)

    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print ''
    print '#################################set-rendezvous-mode#################################'
    try:
        devMgr.SetRendezvousMode(10)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Set rendezvous mode complete"

    print ''
    print '#################################arm-fail-safe#################################'
    failSafeToken = random.randint(0, 2**32)

    try:
        devMgr.ArmFailSafe(1, failSafeToken)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print "Arm fail-safe complete, fail-safe token = " + str(failSafeToken)

    try:
        devMgr.ArmFailSafe(2, failSafeToken)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print "Arm fail-safe complete, fail-safe token = " + str(failSafeToken)

    try:
        devMgr.ArmFailSafe(3, failSafeToken)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print "Arm fail-safe complete, fail-safe token = " + str(failSafeToken)

    print ''
    print '#################################disarm-fail-safe#################################'
    try:
        devMgr.DisarmFailSafe()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Disarm fail-safe complete"

    print ''
    print '#################################reset-config#################################'
    #Reset the device's configuration. <reset-flags> is an optional hex value specifying the type of reset to be performed.
    resetFlags = 0x00FF

    try:
        devMgr.ResetConfig(resetFlags)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Reset config complete"

    print ''
    print '#################################set-auto-reconnect#################################'
    autoReconnect = 1

    try:
        devMgr.SetAutoReconnect(autoReconnect)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Done."

    print ''
    print '#################################set-log-output#################################'
    category = 'detail'
    if (category == 'none'):
        category = 0
    elif (category == 'error'):
        category = 1
    elif (category == 'progress'):
        category = 2
    elif (category == 'detail'):
        category = 3

    try:
        devMgr.SetLogFilter(category)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Done."

    RunWdmClientTest()

    print ''
    print '#################################scan-network#################################'
    try:
        networkType = WeaveDeviceMgr.ParseNetworkType("wifi")
        scanResult = devMgr.ScanNetworks(networkType)
    except WeaveStack.WeaveStackException, ex:
        print ex
    print "ScanNetworks complete, %d network(s) found" % (len(scanResult))
    i = 1
    for net in scanResult:
        print "  Network %d" % (i)
        net.Print("    ")
        i = i + 1
    try:
        networkType = WeaveDeviceMgr.ParseNetworkType("thread")
        scanResult = devMgr.ScanNetworks(networkType)
    except WeaveStack.WeaveStackException, ex:
        print ex
    print "ScanNetworks complete, %d network(s) found" % (len(scanResult))
    i = 1
    for net in scanResult:
        print "  Network %d" % (i)
        net.Print("    ")
        i = i + 1

    print ''
    print  '#################################add-network#################################'
    networkInfo = WeaveDeviceMgr.NetworkInfo(
    networkType = WeaveDeviceMgr.NetworkType_WiFi,
    wifiSSID = "Wireless-1",
    wifiMode = WeaveDeviceMgr.WiFiMode_Managed,
    wifiRole = WeaveDeviceMgr.WiFiRole_Station,
    wifiSecurityType = WeaveDeviceMgr.WiFiSecurityType_None)

    try:
        addResult = devMgr.AddNetwork(networkInfo)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    lastNetworkId = addResult

    print "Add wifi network complete (network id = " + str(addResult) + ")"

    print ''
    print  '#################################update-network#################################'

    # networkInfo = WeaveDeviceMgr.NetworkInfo(networkId=lastNetworkId)

    networkInfo = WeaveDeviceMgr.NetworkInfo(
    networkType = WeaveDeviceMgr.NetworkType_WiFi,
    networkId=lastNetworkId,
    wifiSSID = "Wireless-1",
    wifiMode = WeaveDeviceMgr.WiFiMode_Managed,
    wifiRole = WeaveDeviceMgr.WiFiRole_Station,
    wifiSecurityType = WeaveDeviceMgr.WiFiSecurityType_None)

    try:
        devMgr.UpdateNetwork(networkInfo)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Update network complete"

    print ''
    print  '#################################disable-network#################################'

    try:
        devMgr.DisableNetwork(lastNetworkId)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)

    print "Disable network complete"

    print ''
    print  '#################################enable-network#################################'

    try:
        devMgr.EnableNetwork(lastNetworkId)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print "Enable network complete"

    print ''
    print  '#################################test-network#################################'

    try:
        devMgr.TestNetworkConnectivity(lastNetworkId)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print "Network test complete"

    print ''
    print  '#################################get-networks-without-credentials#################################'

    try:
        # Send a GetNetworks without asking for credentials
        flags = 0
        getResult = devMgr.GetNetworks(flags)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Get networks complete, %d network(s) returned" % (len(getResult))
    i = 1
    for net in getResult:
        print "  Network %d" % (i)
        net.Print("    ")
        i = i + 1

    print ''
    print  '#################################get-networks-with-credentials#################################'

    try:
        # Send a GetNetworks asking for credentials: this is allowed only if the
        # device thinks it's paired. (See the Weave Access Control specification)
        kGetNetwork_IncludeCredentials = 1
        flags = kGetNetwork_IncludeCredentials
        getResult = devMgr.GetNetworks(flags)
    except WeaveStack.WeaveStackException, ex:
        print "expected Device Error: [ Common(00000000):20 ] Access denied"
        print "caught " + str(ex)
        if ex.profileId != 0 or ex.statusCode != 20:
            exit()

    print ''
    print  '#################################remove-network#################################'

    try:
        devMgr.RemoveNetwork(lastNetworkId)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Remove network complete"

    print ''
    print  '#################################create-fabric#################################'

    try:
        devMgr.CreateFabric()
    except WeaveStack.WeaveStackException, ex:
         print 'Already member of fabric'

    print "Create fabric complete"

    print ''
    print  '#################################get-fabric-configure#################################'

    try:
        fabricConfig = devMgr.GetFabricConfig()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Get fabric config complete"
    print "Fabric configuration: " + base64.b64encode(buffer(fabricConfig))

    print ''
    print  '#################################leave-fabric#################################'

    try:
        devMgr.LeaveFabric()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Leave fabric complete"

    print ''
    print '#################################join-existing-fabric#################################'

    try:
        devMgr.JoinExistingFabric(fabricConfig)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Join existing fabric complete"

    print ''
    print '#################################identify#################################'

    try:
        deviceDesc = devMgr.IdentifyDevice()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Identify device complete"
    deviceDesc.Print("  ")

    print ''
    print '#################################get-last-network-provisioning-result#################################'

    try:
        devMgr.GetLastNetworkProvisioningResult()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print  '#################################get-wireless-reg-config#################################'

    try:
        getResult = devMgr.GetWirelessRegulatoryConfig()
    except WeaveDeviceMgr.DeviceManagerException, ex:
        print str(ex)
        exit()

    print "GetWirelessRegulatoryConfig complete"
    getResult.Print("  ")

    print ''
    print  '#################################set-wireless-reg-config#################################'

    try:
        devMgr.SetWirelessRegulatoryConfig(WeaveDeviceMgr.WirelessRegConfig(regDomain="US"))
    except WeaveDeviceMgr.DeviceManagerException, ex:
        print str(ex)
        exit()

    print "SetWirelessRegulatoryConfig complete"

    print ''
    print '#################################ping#################################'

    try:
        devMgr.Ping()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()
    print "Ping complete"

    print ''
    print '#################################register-service#################################'
    try:
        devMgr.RegisterServicePairAccount(0x18B4300100000001, '21245530', base64.b64decode(DUMMY_SERVICE_CONFIG), DUMMY_PAIRING_TOKEN, DUMMY_INIT_DATA)
    except WeaveStack.WeaveStackException, ex:
        print ex
        exit()

    print "Register service complete"

    print ''
    print '#################################update-service#################################'
    try:
        devMgr.UpdateService(0x18B4300100000001, base64.b64decode(DUMMY_SERVICE_CONFIG))
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Update service complete"

    print ''
    print '#################################unregister-service#################################'

    try:
        devMgr.UnregisterService(0x18B4300100000001)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Unregister service complete"
    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()
    print ''
    print '#################################reconnect#################################'

    try:
        devMgr.ReconnectDevice()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print ''
    print '#################################enable-connection-monitor#################################'
    interval = 5000
    timeout = 1000
    try:
        devMgr.EnableConnectionMonitor(interval, timeout)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Connection monitor enabled"

    print ''
    print '#################################disable-connection-monitor#################################'
    try:
        devMgr.DisableConnectionMonitor()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Connection monitor disabled"

    print ''
    print '#################################start-system-test#################################'
    try:
        devMgr.StartSystemTest(WeaveDeviceMgr.SystemTest_ProductList["thermostat"], 1)
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Start system test complete"
    print ''
    print '#################################stop-system-test#################################'
    try:
        devMgr.StopSystemTest()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Stop system test complete"

    print ''
    print '#################################pair-token#################################'
    try:
        tokenPairingBundle = devMgr.PairToken('Test')
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Pair token complete"
    print "TokenPairingBundle: " + base64.b64encode(buffer(tokenPairingBundle))

    print ''
    print '#################################unpair-token#################################'
    try:
        devMgr.UnpairToken()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()

    print "Unpair token complete"


    print ''
    print '#################################close#################################'

    try:
        devMgr.Close()
        devMgr.CloseEndpoints()
    except WeaveStack.WeaveStackException, ex:
        print str(ex)
        exit()
    print "Shutdown complete"

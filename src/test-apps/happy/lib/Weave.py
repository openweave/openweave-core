#!/usr/bin/env python

#
#    Copyright (c) 2015-2017 Nest Labs, Inc.
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

##
#    @file
#       Implements Weave class that wraps around standalone Weave code library.
#

import os
import sys

from happy.Utils import *
from WeaveState import WeaveState


class Weave(WeaveState):
    def __init__(self):
        WeaveState.__init__(self)

        self.weave_happy_conf_path = None
        self.weave_build_path = None
        self.weave_cert_path = None

        self.gateway_tun = "weave-tun0"
        self.service_tun = "service-tun0"

        # default maximum running time for every test case  is 1800 Sec
        self.max_running_time = 1800

    def __check_weave_path(self):
        # Pick weave path from configuration
        if "weave_path" in self.configuration.keys():
            self.weave_happy_conf_path = self.configuration["weave_path"]
            emsg = "Found weave path: %s." % (self.weave_happy_conf_path)
            self.logger.debug("[localhost] Weave: %s" % (emsg))

        # Check if Weave build path is set
        if "abs_builddir" in os.environ.keys():
            self.weave_build_path = os.environ['abs_builddir']
            emsg = "Found weave abs_builddir: %s." % (self.weave_build_path)
            self.logger.debug("[localhost] Weave: %s" % (emsg))

        if self.weave_build_path is not None:
            self.weave_path = self.weave_build_path
        else:
            self.weave_path = self.weave_happy_conf_path

        if self.weave_path is None:
            emsg = "Unknown path to Weave directory (repository)."
            self.logger.error("[localhost] Weave: %s" % (emsg))
            self.logger.info(hyellow("Set weave_path with happy-configuration and try again."))
            sys.exit(1)

        if not os.path.exists(self.weave_path):
            emsg = "Weave path %s does not exist." % (self.weave_path)
            self.logger.error("[localhost] Weave: %s" % (emsg))
            self.logger.info(hyellow("Set correct weave_path with happy-configuration and try again."))
            sys.exit(1)

        if self.weave_path[-1] == "/":
            self.weave_path = self.weave_path[:-1]

    def __get_cmd_path(self, cmd_end):
        cmd_path = self.weave_path + "/" + str(cmd_end)
        if not os.path.exists(cmd_path):
            emsg = "Weave path %s does not exist." % (cmd_path)
            self.logger.error("[localhost] Weave: %s" % (emsg))
            sys.exit(1)
            # return None
        else:
            return cmd_path

    def __setup_weave_cert_path(self):
        if "weave_cert_path" in self.configuration.keys():
            self.weave_cert_path = self.configuration['weave_cert_path']
            self.weave_cert_path = os.path.expandvars(self.weave_cert_path)
            self.weave_cert_path.rstrip('/')

        elif "abs_top_srcdir" in os.environ.keys():
                emsg = "Found weave source path: %s" % os.environ['abs_top_srcdir']
                self.logger.debug("[localhost] Weave: %s" % (emsg))
                self.weave_cert_path = os.environ['abs_top_srcdir'].rstrip('/') + '/certs/development'

        elif "WEAVE_HOME" in os.environ.keys():
                emsg = "Found weave source path: %s" % os.environ['WEAVE_HOME']
                self.logger.debug("[localhost] Weave: %s" % (emsg))
                self.weave_cert_path = '${WEAVE_HOME}/certs/development'

        if not (self.weave_cert_path and
                os.path.isdir(os.path.expandvars(self.weave_cert_path))):
            emsg = "Unable to set up weave_cert_path: unknown path to Weave src directory"
            self.logger.debug("[localhost] Weave: %s" % (emsg))

    def getWeaveConnectionTunnelPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-connection-tunnel")
        return cmd_path

    def getWeaveMessageLayerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestWeaveMessageLayer")
        return cmd_path

    def getWeaveInetLayerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestInetLayer")
        return cmd_path

    def getWeaveInetLayerDNSPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestInetLayerDNS")
        return cmd_path

    def getWeaveInetLayerMulticastPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestInetLayerMulticast")
        return cmd_path

    def getWeavePingPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-ping")
        return cmd_path

    def getWeaveKeyExportPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-key-export")
        return cmd_path

    def getWeaveHeartbeatPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-heartbeat")
        return cmd_path

    def getWeaveWRMPPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestWRMP")
        return cmd_path

    def getWeaveBDXv0ServerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-bdx-server-v0")
        return cmd_path

    def getWeaveBDXv0ClientPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-bdx-client-v0")
        return cmd_path

    def getWeaveBDXServerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-bdx-server-development")
        return cmd_path

    def getWeaveBDXClientPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-bdx-client-development")
        return cmd_path

    def getWeaveSWUServerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-swu-server")
        return cmd_path

    def getWeaveSWUClientPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-swu-client")
        return cmd_path

    def getWeaveTunnelBorderGatewayPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("mock-weave-bg")
        return cmd_path

    def getWeaveTunnelTestServerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestWeaveTunnelServer")
        return cmd_path

    def getWeaveTunnelTestBRPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestWeaveTunnelBR")
        return cmd_path

    def getWeaveMockDevicePath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("mock-device")
        return cmd_path

    def getWeaveDeviceDescriptionClientPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-dd-client")
        return cmd_path

    def getWeaveWeaveDeviceMgrLibPath(self):
        relative_path = os.path.join('..', 'device-manager', 'python', '_WeaveDeviceMgr.so')
        if self.configuration == {} and 'abs_builddir' not in os.environ:
            return os.path.join(os.getcwd(), relative_path)
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path(relative_path)
        return cmd_path

    def getWeaveWDMv0ServerPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestDataManagement")
        return cmd_path

    def getWeaveWdmNextPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("TestWdmNext")
        return cmd_path

    def getWeaveServiceDirPath(self):
        self.__check_weave_path()
        cmd_path = self.__get_cmd_path("weave-service-dir")
        return cmd_path

    def getWeaveCertPath(self):
        self.__setup_weave_cert_path()
        return self.weave_cert_path

    def getBluetoothdPath(self):
        self.__check_weave_path()
        relative_path = os.path.join('..', '..', 'third_party', 'bluez', 'repo', 'src', 'bluetoothd')
        cmd_path = self.__get_cmd_path(relative_path)
        return cmd_path

    def getBtvirtPath(self):
        self.__check_weave_path()
        relative_path = os.path.join('..', '..', 'third_party', 'bluez', 'repo', 'emulator', 'btvirt')
        cmd_path = self.__get_cmd_path(relative_path)
        return cmd_path

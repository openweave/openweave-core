#!/usr/bin/env python

#
#    Copyright (c) 2020 Google, LLC.
#    Copyright (c) 2017 Nest Labs, Inc.
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
#       A03: Calls Weave Echo with wrmp over tunnel between mock device and real service through
#            shared CASE session.
#

import unittest
from test_weave_echo_base import test_weave_echo_base


class test_weave_echo_03(test_weave_echo_base):
    def test_weave_echo_03(self):
        echo_args = {}
        echo_args["wrmp"] = True
        echo_args['test_tag'] = self.__class__.__name__
        echo_args['test_case_name'] = ['Echo-NestService-A03: Calls Weave Echo with wrmp over tunnel between mock device and real service through shared CASE session.']
        print 'test file: ' + self.__class__.__name__

        print "weave echo test A03: to the Core Router using CASE session"
        echo_args["case_shared"] = False
        echo_args["endpoint"] = "ServiceRouter"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Core Router using shared CASE session"
        echo_args["case_shared"] = True
        echo_args["endpoint"] = "ServiceRouter"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Software Update Endpoint using shared CASE session"
        echo_args["endpoint"] = "SoftwareUpdate"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Data Management Endpoint using shared CASE session"
        echo_args["endpoint"] = "DataManagement"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Log Updload Endpoint using shared CASE session"
        echo_args["endpoint"] = "LogUpload"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Service Provisioning Endpoint using shared CASE session"
        echo_args["endpoint"] = "ServiceProvisioning"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Tunnel Endpoint using shared CASE session"
        echo_args["endpoint"] = "Tunnel"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the File Download Endpoint using shared CASE session"
        echo_args["endpoint"] = "FileDownload"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the Bastion Service Endpoint using shared CASE session"
        echo_args["endpoint"] = "BastionService"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

        print "weave echo test A03: to the WOCA Service Endpoint using shared CASE session"
        echo_args["endpoint"] = "WOCA"
        super(test_weave_echo_03, self).weave_echo_base(echo_args)

if __name__ == "__main__":
    unittest.main()

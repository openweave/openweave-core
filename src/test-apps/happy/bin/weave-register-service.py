#!/usr/bin/env python


#
#    Copyright (c) 2015-2018 Nest Labs, Inc.
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
#       A Happy command line utility that generates the service registration cmd.
#
#       The command is executed by instantiating and running ServiceAccountManager class.
#

import getopt
import logging
import logging.handlers
import sys
import set_test_path

import ServiceAccountManager


logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
log_line_format = '%(asctime)s : %(name)s : %(levelname)s :  [%(filename)s : %(lineno)d: '\
                  '%(funcName)s] : %(message)s'
log_line_time_format = '%m/%d/%Y %I:%M:%S %p'
formatter = logging.Formatter(log_line_format, datefmt=log_line_time_format)

if sys.platform == "darwin":
    address = "/var/run/syslog"
else:
    address = "/dev/log"
 
syslog_handler = logging.handlers.SysLogHandler(address=address)
syslog_handler.setFormatter(formatter)
logger.addHandler(syslog_handler)
stream_handler = logging.StreamHandler()
stream_handler.setFormatter(formatter)
logger.addHandler(stream_handler)


if __name__ == "__main__":
    options = ServiceAccountManager.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:t:u:p:",
                                   ["help", "tier=", "username=", "password=" "quiet"])
    except getopt.GetoptError as err:
        print ServiceAccountManager.ServiceAccountManager.__doc__
        print str(err)
        sys.exit("%s: Failed server parse arguments." % (__file__))

    for o, a in opts:
        if o in ("-h", "--help"):
            print ServiceAccountManager.ServiceAccountManager.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-t", "--tier"):
            options["tier"] = a

        elif o in ("-u", "--username"):
            options["username"] = a

        elif o in ("-p", "--password"):
            options["password"] = a

    cmd = ServiceAccountManager.ServiceAccountManager(logger, options)
    cmd.run()

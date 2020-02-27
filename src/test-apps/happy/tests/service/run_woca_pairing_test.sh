#!/bin/bash


#
#    Copyright (c) 2020 Google, LLC.
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
#       Sample code for manually running happy WdmNext tests
#
# Enable case and make tests hit frontdoor
# generate case certificate
# python device-mfg/weave-provisioning-tool/weave-provisioning-tool.py  export-key-files --db 'weave-certificates/simulator-weave-provisioning-db.sqlite'  --outdir /tmp/ --start 18B430AA00000010  --end 18B430AA0000001A

export CASE=1
export USE_SERVICE_DIR=1
export weave_service_address='trouter.weave.integration.nestlabs.com'
export happy_dns='8.8.8.8 172.16.255.1 172.16.255.153 172.16.255.53'
export RESOURCE_IDS='gsrbr1'
export FABRIC_SEED='01474'
export DEVICE_NUMBERS=1

# exit if something fails
set -e

if [[ $# -eq 0 ]]
then
    pairing/test_weave_first_device_pairing_woca.py

else
    for test_to_run in $* ; do python $test_to_run ;  done
fi

echo All tests have run

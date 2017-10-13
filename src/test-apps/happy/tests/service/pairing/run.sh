#!/bin/bash

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

#
# Manually run happy paring test
#
#Enable case and make tests hit frontdoor
#generate case certificate
#python weave-provisioning-tool.py  export-key-files --db 'simulator-weave-provisioning-db.sqlite'  --outdir /tmp/ --start 18B430AA00070100  --end 18B430AA0007010A
#export CASE=1
#export USE_SERVICE_DIR=1
#export weave_service_address='frontdoor.integration.nestlabs.com'
export weave_service_address='tunnel01.weave01.iad02.integration.nestlabs.com'
export happy_dns='8.8.8.8 172.16.255.1 172.16.255.153 172.16.255.53'
export WEAVE_USERNAME='test-it+fabric00601@nestlabs.com'
export WEAVE_PASSWORD='nest-egg'
export RESOURCE_ID='thd1'
export FABRIC_SEED='00601'
export DEVICE_NUMBERS=1
python test_weave_pairing_01.py

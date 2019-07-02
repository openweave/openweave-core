#!/bin/bash


#
#    Copyright (c) 2014-2017 Nest Labs, Inc.
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



program="${builddir}/weave-device-descriptor"

tests=(
    '--vendor 0x235A --product 6 --revision 1 --serial-num 05CA01AC29130044 --mfg-date 2014/03/26 --802-15-4-mac 18:B4:30:00:00:1E:8E:E5 --wifi-mac 18:B4:30:27:83:47 --ssid PROTECT-8EE5 --pairing-code K4H9ET'
    '--vendor 0x235A --product 0x13 --revision 1 --serial-num 15AA01ZZ01160101 --mfg-date 2016/08/05 --device-id 18B4300400000101'
    '--vendor 0x235A --product 6 --revision 1 --serial-num 05CA01AC29130044 --mfg-date 2014/03/26 --802-15-4-mac 18:B4:30:00:00:1E:8E:E5 --wifi-mac 18:B4:30:27:83:47 --ssid-suffix 8EE5 --pairing-code K4H9ET'
)

expectedResults=(
    '1V:235A$P:6$R:1$D:140326$S:05CA01AC29130044$L:18B43000001E8EE5$W:18B430278347$I:PROTECT-8EE5$C:K4H9ET$'
    '1V:235A$P:13$R:1$D:160805$S:15AA01ZZ01160101$E:18B4300400000101$'
    '1V:235A$P:6$R:1$D:140326$S:05CA01AC29130044$L:18B43000001E8EE5$W:18B430278347$H:8EE5$C:K4H9ET$'
)

numTests=${#tests[@]}

for ((testNum = 0; testNum < numTests; testNum++)); do

    testArgs=${tests[${testNum}]}
    expectedResult=${expectedResults[${testNum}]}
    
    actualResult=`${program} encode ${testArgs}`
    
    if [ "${actualResult}" != "${expectedResult}" ]; then
        >&2 echo "${0}: Encode Test ${testNum} FAILED"
        >&2 echo "    Test arguments  : ${testArgs}"
        >&2 echo "    Expected result : ${expectedResult}"
        >&2 echo "    Actual result   : ${actualResult}"
        exit -1
    fi
    
done

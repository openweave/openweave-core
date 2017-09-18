#!/bin/sh


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



program="${top_builddir}/src/lib/support/verhoeff/VerhoeffTest"

${program} test base-10
if test ${?} -ne 0 ; then
    exit -1
fi

${program} test base-16
if test ${?} -ne 0 ; then
    exit -1
fi

${program} test base-32
if test ${?} -ne 0 ; then
    exit -1
fi

${program} test base-36
if test ${?} -ne 0 ; then
    exit -1
fi

exit 0


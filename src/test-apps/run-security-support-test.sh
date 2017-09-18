#!/bin/bash

#
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


SCRIPT_DIR=`DIR=\`dirname "$0"\` && (cd $DIR && pwd )`

WEAVE_ROOT_DIR=`(cd ${SCRIPT_DIR}/../.. && pwd)`

BUILD_TARGET=`${WEAVE_ROOT_DIR}/third_party/nlbuild-autotools/repo/autoconf/config.guess | sed -e 's/[[:digit:].]*$$//g'`

BUILD_TARGET_DIR=${WEAVE_ROOT_DIR}/build/${BUILD_TARGET}

CLASSPATH=\
${BUILD_TARGET_DIR}/src/wrappers/jni/security-support/WeaveSecuritySupport.jar\
:${BUILD_TARGET_DIR}/src/test-apps/wrapper-tests/jni/WeaveJNIWrapperTests.jar

if [ -z "$1" ]; then
    echo "Please specify a test class name"
    exit -1
fi

java -verbose:jni -Xcheck:jni -Dnl.dontDeleteNativeLib=true -cp ${CLASSPATH} $@

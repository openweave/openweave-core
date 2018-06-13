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


# Locate the target-specific Weave build directory. 
if [ -z "${TARGET_BUILD_DIR}" ]; then
    TARGET_BUILD_DIR=${abs_top_builddir}
fi
if [ -z "${TARGET_BUILD_DIR}" ]; then
    if [ -z "${WEAVE_ROOT_DIR}" ]; then
        SCRIPT_DIR=`DIR=\`dirname "$0"\` && (cd ${DIR} && pwd )`
        WEAVE_ROOT_DIR=`(cd ${SCRIPT_DIR}/../.. && pwd)`
    fi
    BUILD_TARGET=`${WEAVE_ROOT_DIR}/third_party/nlbuild-autotools/repo/third_party/autoconf/config.guess | sed -e 's/[[:digit:].]*$$//g'`
    TARGET_BUILD_DIR=${WEAVE_ROOT_DIR}/build/${BUILD_TARGET}
fi

CLASSPATH=\
${TARGET_BUILD_DIR}/src/wrappers/jni/security-support/WeaveSecuritySupport.jar\
:${TARGET_BUILD_DIR}/src/test-apps/wrapper-tests/jni/WeaveJNIWrapperTests.jar

TESTS=$*
if [ -z "${TESTS}" ]; then
    TESTS="ApplicationKeySupportTest HKDFTest PairingCodeSupportTest WeaveKeyExportClientTest WeaveSecuritySupportTest"
fi

for TEST in ${TESTS}; do
    java -Xcheck:jni -Dnl.dontDeleteNativeLib=true -cp ${CLASSPATH} ${TEST}
done

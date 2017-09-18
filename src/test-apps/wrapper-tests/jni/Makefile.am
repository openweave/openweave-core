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

#
#    Description:
#      This file is the GNU automake template for the Nest Weave
#      Java Device Manager library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_WITH_JAVA

JARS                                               = \
    WeaveJNIWrapperTests.jar                         \
    $(NULL)

WeaveJNIWrapperTests_jar_JAVA_SRCS                 = \
    SimpleTest.java                                  \
    HKDFTest.java                                    \
    WeaveSecuritySupportTest.java                    \
    ApplicationKeySupportTest.java                   \
    WeaveKeyExportClientTest.java                    \
    $(NULL)

WeaveJNIWrapperTests_jar_JAVA_CLASSPATHS           = $(top_builddir)/src/wrappers/jni/security-support/WeaveSecuritySupport.jar

WeaveJNIWrapperTests_jar_JFLAGS                    = -source 6 -target 6

include $(abs_top_srcdir)/build/make/java.mk

all-local: all-JARS
install-data-local: install-data-JARS
uninstall-local: uninstall-JARS
mostlyclean-local: mostlyclean-local-JARS

endif # WEAVE_WITH_JAVA

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
